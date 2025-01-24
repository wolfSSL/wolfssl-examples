# wolfSSL X509 Store Example

This directory contains:

A simple example of using the X509_STORE compatibility layer API with wolfSSL.

## Compiling and Running the Example

```
$ cd wolfssl
$ ./autogen.sh                # If downloaded from github
$ ./configure --enable-opensslall
$ make
$ sudo make install
$ sudo ldconfig # if wanting to update links and cache of recent shared library
```

```
$ cd wolfssl-examples
$ cd certstore
$ make
$ ./certverify
```

