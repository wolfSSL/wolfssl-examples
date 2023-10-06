# wolfSSL CertManager Example

This directory contains:

A simple example of using the wolfSSL CertManager to verify a certificate
in a standalone manner, separate from an SSL/TLS connection.

## Compiling and Running the Example

```
$ cd wolfssl
$ ./autogen.sh                # If downloaded from github
$ ./configure --enable-crl    # CRL is optional.
$ make all
$ make check
$ sudo make install
```

```
$ cd wolfssl-examples
$ cd certmanager
$ make all
$ ./certverify
```

