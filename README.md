wolfSSL Example Applications
============================

This repository contains example applications, written in C, which
demonstrate how to use the wolfSSL lightweight SSL/TLS library for secure
communication.

Each directory represents a unique topic (SSL/TLS, DTLS, PSK, etc.) and
contains a Makefile as well as a simple tutorial on the given topic.

## Current Examples


#### android (Android Studio and NDK Examples)

This directory contains examples that demonstrate using wolfSSL,
wolfSSL JNI/JSSE, and wolfCrypt JNI/JCE on the Android platform. Examples
use either Android Studio with Gradle or the Android NDK standalone toolchain.

Please see the [android/README.md](android/README.md) for further usage and 
details.

<br />

#### BTLE

This directory contains examples for securing a Bluetooth Low Energy Link (BTLE).
BTLE packets are small and throughput is low, so these examples demonstrate a
way to exchange data securely without BTLE pairing.

Please see the [btle/README.md](btle/README.md) for further usage and details.

<br />

#### can-bus

This directory contains an example echo server/client using wolfSSL over
CAN bus with an ISO-TP transport layer. It can be used with a virtual CAN bus
in Linux.

Please see the [can-bus/README.md](can-bus/README.md) for further usage and details.

<br />

#### certfields (X509 field extraction)

This directory contains an example that demonstrate using wolfSSL
to read a DER encoded certificate and extract the public key and
subject name information.

Please see the [certfields/README.md](certfields/README.md) for further usage
and details.


<br />

#### certgen (wolfSSL Certificate Generation)

This directory contains examples that demonstrate using wolfSSL to
generate and sign certificates.

Please see the [certgen/README.md](certgen/README.md) for further usage and
details.


<br />

#### certmanager (wolfSSL CertManager)

This directory contains examples that demonstrate using 
CertManager (Certificate Manager) functionality.

Please see the [certmanager/README.md](certmanager/README.md) for further usage
and details.


<br />

#### crypto (wolfCrypt Examples)

This directory contains examples that demonstrate using the wolfCrypt functionality
to encrypt files with different algorithms (aes, 3des, etc.)

Please see the [crypto/README.md](crypto/README.md) for further usage and details.


<br />

#### custom-io-callbacks (wolfSSL Custom IO Callbacks)

This directory contains examples that demonstrate how the custom IO callbacks
can be used to 4 facilitate a TLS connection using any medium.

Please see the [custom-io-callbacks/README.md](custom-io-callbacks/README.md) 
for further usage and details.


<br />

#### DTLS (Datagram TLS)

This directory contains examples of using DTLS, with client and server
examples demonstrating UDP, DTLS, non-blocking, session resumption,
and multi-threading.

When compiling wolfSSL for use with these examples, wolfSSL will need to be
compiled with DTLS support:

```
cd wolfssl-[version]
./configure --enable-dtls
```

Examples in this directory may be compiled using:

```
cd ./dtls
make
```

Please see the [dtls/README.md](dtls/README.md) for further usage and details.


<br />

#### ecc (Elliptic Curve Cryptography)

This directory contains examples that demonstrate the various use-cases of 
wolfcrypt ECC.

Please see the [ecc/README.md](ecc/README.md) for further usage and details.

<br />

#### ESP32 (Espressif)

This directory contains examples for the Espressif ESP32 chips.

Please see the [ESP32/README.md](ecc/README.md) for further usage and details.


<br />

#### embedded (Embedded Systems)

This directory contains examples that demonstrate TLS client/servers communicating
through buffers and using sockets.

Please see the [embedded/README.md](embedded/README.md) for further usage and
details.


<br />

#### hash (wolfCrypt Hash Examples)

This directory contains examples that demonstrate how to hash an input file using 
wolfCrypt.

Please see the [hash/README.md](hash/README.md) for further usage and details.


<br />

#### java (wolfJSSE Examples)

This directory contains examples that demonstrate HTTPS URL use with wolfJSSE
and example keystores.

Please see the [java/README.md](java/README.md) for further usage and details.


<br />

#### mynewt (Apache Mynewt Examples)

This directory contains examples that demonstrate using wolfSSL with Apache 
Mynewt OS.

Please see the [mynewt/README.md](mynewt/README.md) for further usage and 
details.


<br />

#### picotcp (picoTCP Examples)

This directory contains a TLS server created by using picoTCP via wolfSSL 
custom callbacks.

Please see the [picotcp/README.md](picotcp/README.md) for further usage and 
details.


<br />

#### picotcp (picoTCP Examples)

This directory contains a TLS server created by using picoTCP via wolfSSL 
custom callbacks.

Please see the [picotcp/README.md](picotcp/README.md) for further usage and 
details.


<br />

#### pk (Public-Key)

This directory contains examples that demonstrate various wolfCrypt public-key 
functionality (storing and loading keys after generation, extracting public key
from private key, etc.).

Please see the [pk/README.md](pk/README.md) for further usage and details.


<br />

#### pkcs11 (PKCS #11)

This directory contains examples of using wolfSSL's PKCS #11 feature and a TLS
server example using a PKCS 11 based key.

Please see the [pkcs11/README.md](pkcs11/README.md) for further usage and details.


<br />

#### pkcs7 (PKCS #7)

This directory contains example applications that demonstrate usage of the 
wolfCrypt PKCS#7/CMS API, included in the [wolfSSL embedded SSL/TLS library].

It includes examples of several different content types (EncryptedData, SignedData,
EnvelopedData, CompressedData), and demonstrates both signing/encrypting and 
verifying/decrypting operations.

Please see the [pkcs7/README.md](pkcs7/README.md) for further usage and details.


<br />

#### PSK (Pre-Shared Keys)

This directory contains examples of using PSK, with client and server examples 
demonstrating TCP/IP, PSK, non-blocking, session resumption, and multi-threading.

When compiling wolfSSL for use with these examples, wolfSSL will need to be
compiled with PSK support:

```
cd wolfssl-[version]
./configure --enable-psk
```

Examples in this directory may be compiled using:

```
cd ./psk
make
```

Please see the [psk/README.md](psk/README.md) for further usage and details.


<br />

#### riot-os-posix-lwip (RIOT-OS)

This directory contains examples that demonstrate how to use wolfSSL TLS sockets
over RIOT-OS POSIX sockets.

Please see the [riot-os-posix-lwip/README.md](riot-os-posix-lwip/README.md) for
further usage and details.


<br />

#### RT1060 (i.MX RT1060-EVK)

This directory contains a wolfCrypt benchmark test application for i.MX RT1060-EVK.

Please see the [RT1060/README.md](RT1060/README.md) for further usage and details.


<br />

#### NXP SE050

This directory contains example applications for NXP's SE050. The examples
have been tested on a Raspberry Pi with SE050 EdgeLock.

Please see the [SE050/README.md](SE050/README.md) for further details.


<br />

#### SGX_Linux (Linux Enclave)

This directory contains an example application, written in C, which demonstrates
how to link the wolfSSL lightweight SSL/TLS library with a simple Enclave using 
Linux. The example has been tested with Ubuntu 16.04.

Please see the [SGX_Linux/README.md](SGX_Linux/README.md) for further usage and 
details.


<br />

#### SGX_Windows (Windows Enclave)

This directory contains an example application, written in C++, which demonstrates
how to link the wolfSSL lightweight SSL/TLS library with a simple Enclave using
Windows.

Please see the [SGX_Windows/README.md](SGX_Windows/README.md) for further usage 
and details.


<br />

#### signature (Sign and Verify Examples)

This directory contains examples that demonstrate using wolfSSL to sign and
verify binary data (supports RSA and ECC for signing and MD2, MD4, MD5, SHA, 
SHA224, SHA256, SHA384 and SHA512).

Please see the [signature/README.md](signature/README.md) for further usage and 
details.


<br />

#### tirtos_ccs_examples (TI-RTOS)

This directory contains a client/server examples that demonstrates using wolfSSL
in a TI-RTOS ecosystem.

Please see the [tirtos_ccs_examples/README.md](tirtos_ccs_examples/README.md) for
further usage and details.


<br />

#### TLS

This directory contains examples of using SSL/TLS, with client and server
examples demonstrating TCP/IP, SSL/TLS, non-blocking, session resumption, and
multi-threading.

Examples in this directory may be compiled using:

```
cd ./tls
make
```

Please see the [tls/README.md](tls/README.md) for further usage and details.


<br />

#### utasker (uTasker wolfSSL Example Tasks)

This directory contains example uTasker client and server tasks  that
demonstrate using wolfSSL with the uTasker stack. These have been tested on
the uTasker Simulator.

Please see the [utasker/README.md](utasker/README.md) for further usage and
details.


<br />

## Notes


When necessary, examples will use the example certificates and keys located
in the ./certs directory. These certificates and keys have been pulled in from
the main wolfSSL repository.

To generate your own cert text, see the [DER to C script](https://github.com/wolfSSL/wolfssl/blob/master/scripts/dertoc.pl).

<br />

## Support

Please contact wolfSSL at support@wolfssl.com with any questions, bug fixes,
or suggested feature additions.

