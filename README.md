wolfSSL Example Applications
============================

This repository contains example applications, written in C, which
demonstrate how to use the wolfSSL lightweight SSL/TLS library for secure
communication.

Each directory represents a unique topic (SSL/TLS, DTLS, PSK, etc.) and
contains a Makefile as well as a simple tutorial on the given topic.

## Current Examples

#### utasker (uTasker wolfSSL Example Tasks)

This directory contains example uTasker client and server tasks  that
demonstrate using wolfSSL with the uTasker stack. These have been tested on
the uTasker Simulator.

Please see the README.md in utasker/ for further usage and details.

#### android (Android NDK Examples)

This directory contains examples that demonstrate using wolfSSL and wolfSSLJNI
on the Android platform, using the Android NDK toolchain.

Please see the README.md in android/ for further usage and details.

#### certmanager (wolfSSL CertManager)

This directory contains examples that demonstrate using the wolfSSL
CertManager (Certificate Manager) functionality.

Please see the README.md in certmanager/ for further usage and details.

#### clu (wolfSSL Command Line Utility)

This is a tool to provide command line access to wolfcrypt cryptographic
libraries. wolfSSL command line utility will allow users to encrypt or decrypt
a user specified file to any file name and extension.

Please see the README.md in clu/ for further usage and details.

###### Unique feature to wolfSSL CLU
The decision to allow for unique file extensions was prompted by automated
tools available for brute forcing files. It will not provide extra security
cryptographically however it will force attackers to check the header
information on every single brute force attempt. This will provide further
frustration and an extra step in any attempt to brute force a file encrypted
with our utility.

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

#### PSK (Pre-Shared Keys)

This directory contains examples of using PSK, with client and server examples
demonstrating TCP/IP, PSK, non-blocking, session resumption, and
multi-threading.

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

#### SSL/TLS

This directory contains examples of using SSL/TLS, with client and server
examples demonstrating TCP/IP, SSL/TLS, non-blocking, session resumption, and
multi-threading.

Examples in this directory may be compiled using:

```
cd ./tls
make
```

## Notes

When necessary, examples will use the example certificates and keys located
in the ./certs directory. These certificates and keys have been pulled in from
the main wolfSSL repository.

## Support

Please contact wolfSSL at support@wolfssl.com with any questions, bug fixes,
or suggested feature additions.

