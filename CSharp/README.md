# C# Wrapper Examples

This directory contains examples that demonstrate using the C# wrapper.

These assume the use of `wolfssl\wrapper\CSharp\wolfSSL_CSharp.sln`.

## wolfSSL-TLS-pq

wolfSSL Server and Client using PQC algorithms (ML-KEM / ML-DSA).

### Build Options

`wrapper/CSharp/user_settings.h` already enables these, so a wolfSSL built
against it needs nothing added:

```
HAVE_MLKEM
WOLFSSL_HAVE_MLKEM
WOLFSSL_DTLS_CH_FRAG
WOLFSSL_HAVE_MLDSA
WOLFSSL_SHAKE128
WOLFSSL_SHAKE256
```

On Linux that is:

```sh
cd wolfssl
./configure --enable-usersettings CPPFLAGS=-I$PWD/wrapper/CSharp
make && sudo make install
```

The Visual Studio `wolfssl` project needs the same defines added to it.
The `wolfSSL_CSharp` project needs none: the wrapper compiles the ML-KEM and
ML-DSA bindings unconditionally.

### wolfSSL-TLS-pq-Server

wolfSSL Server using ML-DSA-87.

### wolfSSL-TLS-pq-ServerThreaded

Threaded version of `wolfSSL-TLS-pq-Server`.

### wolfSSL-TLS-pq-Client

wolfSSL Client using ML-KEM-1024/ML-DSA-87.