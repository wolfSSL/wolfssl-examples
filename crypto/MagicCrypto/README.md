# Overview
The MagicCrypto library is created by [Dream Security](https://dreamsecurity.com) which contains a certified implementation of the [ARIA cipher](https://en.wikipedia.org/wiki/ARIA_(cipher)) that is used by the South Korean government. This example makes use of the wolfSSL bindings to create a TLS 1.2 connection.

# Compiling
You will need to compile wolfSSL with:
./configure --enable-ariagcm --enable-cryptocb && make install

You will need to have the MagicCrypto headers and library in the wolfSSL source directory. You may need to apply the patch file to clear up some of the compiler warnings.

Once the wolfSSL library in installed, you can run `make` in this folder to generate the sample client and server applications. You may need to modify the Makefile to point to the MagicCrypto includes directory.

# Usage
The sample applications depend on using the certificates found in the wolfSSL source directory. You should execute them from the folder containing the 'certs' folder. Alternatively, you can modify the `#define` in 'common.h'.

To run, simply start `./server` and `./client 127.0.0.1`.
