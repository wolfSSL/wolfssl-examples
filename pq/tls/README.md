# wolfSSL Post-Quantum Cryptography Example

This directory contains:

- A server application that perform a completely quantum-safe TLS 1.3
  connection.
- A client application that perform a completely quantum-safe TLS 1.3
  connection with the server above.
- An STM32CubeIDE project for doing quantum-safe TLS 1.3 connection over UART
  and some applications that run on the Linux side to connect with it.

# Prerequisites

Build wolfSSL with support for ML-KEM (Kyber) and ML-DSA (Dilthium

```
./autogen.sh
./configure --enable-kyber --enable-dilithium
make all
sudo make install
sudo ldconfig
```
 
## Building the Server and Client

```
$ make
```

## Quantum safe TLS 1.3 Connection

`client-pq-tls13` will connect with `server-pq-tls13` via a completely quantum-
safe connection. Authentication will be done via the ML-DSA signature scheme.
Ephemeral key establishment will be done via ML-KEM. Both are standardized by
NIST. Please see
https://github.com/wolfSSL/osp/tree/master/oqs/README.md for instructions about
certificate generation. For your convenience, certificates and keys have already
been generated and are in this repo's `certs` directory.

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
