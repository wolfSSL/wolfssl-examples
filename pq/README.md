# wolfSSL Post-Quantum Cryptography Example

This directory contains:

- A simple example of using the wolfSSL CertManager to verify a falcon
  certificate chain in a standalone manner, separate from an SSL/TLS connection.
- A simple example of using wolfCrypt APIs to sign a message with a SPHINCS+
  private key and verify that message using the corresponding SPHINCS+ public
  key from a an X.509 certificate.
  certificate chain in a standalone manner, separate from an SSL/TLS connection.
- A server application that perform a completely quantum-safe TLS 1.3
  connection.
- A client application that perform a completely quantum-safe TLS 1.3
  connection with the server above.
- An STM32CubeIDE project for doing quantum-safe TLS 1.3 connection over UART
  and some applications that run on the Linux side to connect with it.
- A simple example of using wolfCrypt LMS hooks to generate and sign a message
  with configurable LMS-HSS parameters. Requires wolfssl with --enable-lms=yes
  and --with-liblms=yes.

# Prerequisites

Please see the wolfSSL repo's INSTALL file:

https://github.com/wolfSSL/wolfssl/blob/master/INSTALL

Item 15 (Building with liboqs for TLS 1.3 [EXPERIMENTAL]) has instructions on
how to configure and build:

- liboqs
- wolfssl
- patched OQS's OpenSSL fork

The LMS sign verify example requires that hash-sigs has been built, and
wolfSSL has been built with LMS/HSS support enabled.  Please see Item 17
in the wolfSSL repo's INSTALL file.

## Building the Applications

```
$ make
```

## Verification of OQS Falcon Certificates

The `generate_falcon_chains.sh` script in the `oqs` directory in the `osp` repo
will allow you to use a patched version of OQS's OpenSSL fork in order to
generate a self-signed CA certificate and entity certificate that uses the
Falcon signature scheme. In the OpenSSL directory, run the script to generate
the certificates and then copy them into this directory. Please see
https://github.com/wolfSSL/osp/tree/master/oqs/README.md for further
instructions about certificate generation.

Once that is complete, execute `falcon_certverify`:

```
$ ./falcon_certverify
```

## Signing and Verifying a Message with SPHINCS+

The `generate_sphincs_chains.sh` script in the `oqs` directory in the `osp` repo
will allow you to use a patched version of OQS's OpenSSL fork in order to
generate a self-signed CA certificate and entity certificate that uses the
SPHINCS+ signature scheme. In the OpenSSL directory, run the script to generate
the certificates and then copy them into this directory. Please see
https://github.com/wolfSSL/osp/tree/master/oqs/README.md for further
instructions about certificate generation.

Once that is complete, execute `sphincs_sign_verify`:

```
$ ./sphincs_sign_verify
```

## Quantum safe TLS 1.3 Connection

`client-pq-tls13` will connect with `server-pq-tls13` via a completely quantum-
safe connection. Authentication will be done via the FALCON signature scheme.
Ephemeral key establishment will be done via kYBER KEM. Both are NIST PQC
competition round 3 finalists. Please see
https://github.com/wolfSSL/osp/tree/master/oqs/README.md for further
instructions about certificate generation. 

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


## Signing and Verifying a Message with LMS/HSS

Run the `lms_sign_verify` example without options to see its help and usage:
```sh
$./lms_sign_verify
usage:
  ./lms_sign_verify <levels> <height> <winternitz> [num signatures]

examples:
  ./lms_sign_verify 1 5 1
  ./lms_sign_verify 3 5 4 100
  ./lms_sign_verify 2 10 2 0

description:
...
```
