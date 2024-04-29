# wolfSSL Cert Verification Examples

This directory contains:

A simple example of using the low level wolfSSL APIs to verify a certificate
in a standalone manner, separate from an SSL/TLS connection.

## Compiling and Running the Example

```
$ cd wolfssl
$ ./autogen.sh                # If downloaded from github
$ ./configure '--enable-cryptonly' '--enable-singlethreaded' 'CFLAGS=-DWOLFSSL_SMALL_CERT_VERIFY'
$ make all
$ make check
$ sudo make install
```

```
$ cd wolfssl-examples
$ cd certvfy
$ make all
$ ./certvfy
$ ./certsigvfy
$ ./sigvfycert
```

