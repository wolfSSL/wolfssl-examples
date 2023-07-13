# wolfSSL Post-Quantum Cryptography Stateful Hash-based Signatures Example

This directory contains:

- A simple example that uses wolfCrypt LMS/HSS hooks to sign and verify a message
  with configurable LMS/HSS parameters. Requires wolfssl with `--enable-lms=yes`
  and `--with-liblms=<path to hash-sigs install>`.

# Prerequisites

The LMS sign verify example requires that hash-sigs has been built, and
wolfSSL has been built with LMS/HSS support enabled.  Please see Item 17
in the wolfSSL repo's INSTALL file.

https://github.com/wolfSSL/wolfssl/blob/master/INSTALL

## Building the Applications

Configure the Makefile to point to your hash-sigs install:

```
HSS_INC  = <path to hss install>
```

```
HSS_LIB         = <path to hss_lib_thread.a>
```

Then build:

```
$ make
```

## Signing and Verifying a Message with LMS/HSS

This example will generate an LMS/HSS key pair with L=levels, H=height, and
W=Winternitz parameters, then sign and verify a given number of signatures.
It will also print the signature size, the total number of signatures, and
the public and private key lengths.

While LMS/HSS have small public and private keys, and fast signing and
verifying, the initial key generation can be quite slow and intensive,
especially for larger heights and Winternitz parameters.

LMS/HSS signature systems have a finite number of one-time signatures (OTS).
The number of available signatures is
  N = 2 ** (levels * height)

The supported parameter values are those in RFC8554:
- levels = {1..8}
- height = {5, 10, 15, 20, 25}
- Winternitz = {1, 2, 4, 8}

To see the help and usage, run the program without options:
```sh
$./lms_example
usage:
  ./lms_example <levels> <height> <winternitz> [num signatures]

examples:
  ./lms_example 1 5 1
  ./lms_example 3 5 4 100
  ./lms_example 2 10 2 0

description:
...
```
