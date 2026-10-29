# Description

ML-KEM key generation, encapsulation and decapsulation routed through the
wolfCrypt crypto callback (cryptocb) interface. This is how a secure element or
other offload target implements ML-KEM for TLS 1.3: wolfSSL dispatches the KEM
operations under `WC_ALGO_TYPE_PK` to a registered callback, which selects
ML-KEM with `WC_PQC_KEM_TYPE_MLKEM`.

The callback handles three operation types:

- `WC_PK_TYPE_PQC_KEM_KEYGEN` (`info->pk.pqc_kem_kg`) - generate a key pair. In
  TLS 1.3 the client does this for its key_share.
- `WC_PK_TYPE_PQC_KEM_ENCAPS` (`info->pk.pqc_encaps`) - encapsulate to a public
  key. In TLS 1.3 the server does this.
- `WC_PK_TYPE_PQC_KEM_DECAPS` (`info->pk.pqc_decaps`) - decapsulate a
  ciphertext. In TLS 1.3 the client does this.

A TLS 1.3 client therefore needs KEYGEN and DECAPS; a server needs ENCAPS.

The example's callback delegates back to the wolfCrypt software implementation
so it runs without hardware. A real device replaces each case body with its own
calls and keeps the private key on-chip. It is registered with
`wc_CryptoCb_RegisterDevice()`; for TLS, associate it with a context using
`wolfSSL_CTX_SetDevId()`.

Requires wolfSSL built with:

```sh
$ ./autogen.sh
$ ./configure --enable-cryptocb --enable-mlkem
$ make
$ sudo make install
$ sudo ldconfig
```

Build the example:

```sh
$ make
```

# Usage

```
$ ./ml_kem_cryptocb
```

The ML-KEM keys are initialized with a callback device id, so key generation,
encapsulation and decapsulation all dispatch to the callback. The program
prints the per-operation callback counts and confirms both sides derived the
same shared secret.

# Related examples

- `pq/tls_cryptocb` - the same ML-KEM callback (`pq/mlkem-cryptocb.c`) offloading
  the key_share in a TLS 1.3 handshake.
- `pq/ml_kem` - the same ML-KEM exchange in software, without a crypto callback.
- `pq/ml_dsa_cryptocb` - the ML-DSA signature counterpart.
