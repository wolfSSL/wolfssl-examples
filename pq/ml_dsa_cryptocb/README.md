# Description

ML-DSA key generation, signing and verification routed through the wolfCrypt
crypto callback (cryptocb) interface. wolfSSL dispatches the signature
operations under `WC_ALGO_TYPE_PK` to a registered callback, which selects
ML-DSA with `WC_PQC_SIG_TYPE_MLDSA`.

The callback handles three operation types:

- `WC_PK_TYPE_PQC_SIG_KEYGEN` (`info->pk.pqc_sig_kg`) - generate a key pair.
- `WC_PK_TYPE_PQC_SIG_SIGN` (`info->pk.pqc_sign`) - sign a message.
- `WC_PK_TYPE_PQC_SIG_VERIFY` (`info->pk.pqc_verify`) - verify a signature.

The example signs and verifies with an empty context (FIPS 204 pure signing);
a pre-hash request falls back to software. The callback delegates back to the
wolfCrypt software implementation so it runs without hardware. A real device
replaces each case body with its own calls and keeps the private key on-chip.
It is registered with `wc_CryptoCb_RegisterDevice()`; for TLS, associate it
with a context using `wolfSSL_CTX_SetDevId()`.

Requires wolfSSL built with:

```sh
$ ./autogen.sh
$ ./configure --enable-cryptocb --enable-dilithium
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
$ ./ml_dsa_cryptocb
```

The ML-DSA key is initialized with a callback device id, so key generation,
signing and verification all dispatch to the callback. The program prints the
per-operation callback counts and confirms the signature verified.
