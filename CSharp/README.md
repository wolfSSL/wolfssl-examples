# C# Wrapper Examples

This directory contains examples that demonstrate using the C# wrapper.

These assume the use of `wolfssl\wrapper\CSharp\wolfSSL_CSharp.sln`.

## wolfSSL-TLS-pq

wolfSSL Server and Client using PQC algorithms (ML-KEM / ML-DSA).

### Build Options

The following build options need to be added.

#### for `wolfssl` Project

```
HAVE_MLKEM
WOLFSSL_WC_MLKEM
WOLFSSL_HAVE_MLKEM
WOLFSSL_DTLS_CH_FRAG
HAVE_DILITHIUM
WOLFSSL_WC_DILITHIUM
WOLFSSL_SHAKE128
WOLFSSL_SHAKE256
```

#### for `wolfSSL_CSharp` Project

```
HAVE_MLKEM
HAVE_MLDSA
```

If you want to execute `wolfCrypt-Test` Project as well, add these options to `wolfCrypt-Test` Project.

### wolfSSL-TLS-pq-Server

wolfSSL Server using ML-DSA-87.

### wolfSSL-TLS-pq-ServerThreaded

Threaded version of `wolfSSL-TLS-pq-Server`.

### wolfSSL-TLS-pq-Client

wolfSSL Client using ML-KEM-1024/ML-DSA-87.