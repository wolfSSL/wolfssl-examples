# wolfSSL X509 Store Example

This directory contains:

A simple example of using the X509_STORE compatibility layer API with wolfSSL.

## Compiling and Running the Example

```
$ cd wolfssl
$ ./autogen.sh                # If downloaded from github
$ ./configure --enable-opensslall--enable-crl    # CRL is optional.
$ make
$ sudo make install
```

```
$ cd wolfssl-examples
$ cd certstore
$ make
$ ./certverify
```

