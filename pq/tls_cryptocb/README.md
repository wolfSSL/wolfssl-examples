# Description

TLS 1.3 client and server that offload the ML-KEM key exchange (the key_share)
through the wolfCrypt crypto callback (cryptocb) interface. This is the path a
secure element uses to handle post-quantum key exchange for TLS 1.3.

wolfSSL dispatches the KEM operations under `WC_ALGO_TYPE_PK` to the registered
callback (`pq/mlkem-cryptocb.c`, shared with `pq/ml_kem_cryptocb`), selecting ML-KEM with
`WC_PQC_KEM_TYPE_MLKEM`:

- the client generates its key pair (`WC_PK_TYPE_PQC_KEM_KEYGEN`) and recovers
  the shared secret (`WC_PK_TYPE_PQC_KEM_DECAPS`);
- the server encapsulates to the client's public key
  (`WC_PK_TYPE_PQC_KEM_ENCAPS`).

The callback is shared by both programs. It is registered with
`wc_CryptoCb_RegisterDevice()` and bound to the context with
`wolfSSL_CTX_SetDevId()`; the devId then flows into the handshake's ML-KEM keys
automatically. The client requests the ML-KEM-768 hybrid group with
`wolfSSL_UseKeyShare(ssl, WOLFSSL_SECP256R1MLKEM768)` (ECC P-256 + ML-KEM-768);
its ML-KEM half is what dispatches to the callback. This hybrid is on by
default. For a pure, non-hybrid group such as `WOLFSSL_ML_KEM_768`, build
wolfSSL with `--enable-tls-mlkem-standalone` and use that group instead.

The callback delegates back to the wolfCrypt software implementation so the
example runs without hardware. A real device replaces each case body with its
own calls and keeps the private key on-chip.

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

In one terminal start the server:

```sh
$ ./server-tls13-mlkem
```

In another terminal run the client:

```sh
$ ./client-tls13-mlkem 127.0.0.1
```

Both print the negotiated group and the per-operation callback counts, proving
the ML-KEM key exchange was offloaded to the callback.

# Related examples

- `pq/ml_kem_cryptocb` - the same ML-KEM callback (`pq/mlkem-cryptocb.c`) driven
  directly from wolfCrypt, without TLS.
- `pq/tls` - the same TLS 1.3 ML-KEM hybrid handshake in software, without a
  crypto callback.
- `ccb_vaultic` - a real secure-element (SealSQ VaultIC) crypto callback;
  classical ECC/RSA today, the pattern a device backend fills in for ML-KEM.
