# wolfSSL Post-Quantum Cryptography Example

This directory contains:

- A simple example of using the wolfSSL CertManager to verify a falcon
  certificate chain in a standalone manner, separate from an SSL/TLS connection.
- A server application that perform a completely quantum-safe TLS 1.3
  connection.
- A client application that perform a completely quantum-safe TLS 1.3
  connection with the server above.

# Prerequisites

Support for the quantum-safe signature scheme FALCON is new and has not made it
into a release yet. You can try these examples against the following github PR:

https://github.com/wolfSSL/wolfssl/pull/4530

Please see wolfssl/INSTALL for instructions on how to build and install the
Open Quantum Safe project's liboqs. For a quick start, you can go into wolfssl
and do the following:

```
$ ./autogen.sh
$ ./configure --with-liboqs
$ make all check
# sudo make install
```

Once you have built that, you will then need to build the Open Quantum Safe
project's OpenSSL. Instructions for downloading and building their OpenSSL fork
can be found here:

https://github.com/open-quantum-safe/openssl/releases/tag/OQS-OpenSSL_1_1_1-stable-snapshot-2021-08

Note that installation of the OpenSSL fork is NOT neccessary.

## Building the Applications

```
$ make
```

## Verification of OQS Falcon Certificates

The `generate_falcon_chains.sh` script will allow you to use the OQS project's
OpenSSL in order to generate a self-signed CA certificate and entity
certificate that uses the Falcon signature scheme. In the OpenSSL directory,
run the script to generate the certificates and then copy them into this
directory.

Once that is complete, execute `falcon_certverify`:

```
$ ./falcon_certverify
```

## Quantum safe TLS 1.3 Connection

`client-pq-tls13` will connect with `server-pq-tls13` via a completely quantum-
safe connection. Authentication will be done via the FALCON signature scheme.
Ephemeral key establishment will be done via kYBER KEM. Both are NIST PQC
competition round 3 finalists.

In a terminal, execute the server:

```sh
./server-pq-tls13
```

In another terminal, execute the client:

```sh
./client-pq-tls13 127.0.0.1
```

The client will be prompted for a message to send to the server. Once you see
this prompt, a quantum-safe connection has already been established. Use the
client to send the message "shutdown" in order to end the execution of the
server.
