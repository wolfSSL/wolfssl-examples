# OCSP Stapling Example with wolfSSL

This directory contains a standalone example demonstrating OCSP stapling with dynamic certificate selection and verification. Both a client and a server are provided, along with the necessary certificates and a Makefile for building the example. The certificates are taken from the wolfSSL test suite.

## Directory Structure

- `ocsp-server.c` — Example TLS server with OCSP stapling support.
- `ocsp-client.c` — Example TLS client that verifies OCSP staples.
- `Makefile` — Build instructions for the example programs.
- `client-certs/` — CA and intermediate certificates for client verification.
- `server-certs/` — Server certificate and private key.
- `responder-certs/` — OCSP responder certificate, key, and index file.

## Prerequisites

- wolfSSL library installed (headers and libraries in `/usr/local` by default).
- OpenSSL (for running a local OCSP responder).
- GNU Make and GCC.

## Building

For this example, you need to build wolfSSL with OCSP and session certificate support. The following configuration options are required:

```sh
./configure --enable-ocsp --enable-ocspstapling --enable-ocspstapling2 --enable-cert-setup-cb --enable-sessioncerts
make
make install
```

To build both the server and client, simply run:

```sh
make
```

This will produce two binaries: `ocsp-server` and `ocsp-client`.

## Running the Example

### 1. Start the OCSP Responder

From the `stapling` directory, run:

```sh
make responder
```

### 2. Start the Server

```sh
./ocsp-server
```

### 3. Run the Client

In a separate terminal, run either:

```sh
./ocsp-client --tls12
```
or
```sh
./ocsp-client --tls13
```

## Notes

- The server listens on `127.0.0.1:11111`.
- The OCSP responder listens on `127.0.0.1:22221`.
- Certificates are pre-generated for demonstration purposes.
- The client and server demonstrate both automatic and manual OCSP staple verification.
