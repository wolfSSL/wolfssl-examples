wolfSSL Example Applications
============================

This repository contains example applications, written in C, which 
demonstrate how to use the CyaSSL lightweight SSL/TLS library for secure 
communication.

Each directory represents a unique topic (SSL/TLS, DTLS, PSK, etc.) and 
contains a Makefile as well as a simple tutorial on the given topic.

## Current Examples

#### DTLS (Datagram TLS)

This directory contains examples of using DTLS, with client and server 
examples demonstrating UDP, DTLS, non-blocking, session resumption, 
and multi-threading.

When compiling CyaSSL for use with these examples, CyaSSL will need to be 
compiled with DTLS support:

```
cd cyassl-[version]
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

When compiling CyaSSL for use with these examples, CyaSSL will need to be 
compiled with PSK support:

```
cd cyassl-[version]
./configure --enable-psk
```

Examples in this directory may be compiled using:

```
cd ./dtls
make
```

#### SSL/TLS

This directory contains examples of using SSL/TLS, with client and server 
examples demonstrating TCP/IP, SSL/TLS, non-blocking, session resumption, and 
multi-threading.

Examples in this directory may be compiled using:

```
cd ./dtls
make
```

## Notes

When necessary, examples will use the example certificates and keys located 
in the ./certs directory. These certificates and keys have been pulled in from 
the main CyaSSL repository.

## Support

Please contact wolfSSL at support@wolfssl.com with any questions, bug fixes, 
or suggested feature additions.

