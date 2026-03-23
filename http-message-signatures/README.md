# RFC 9421 HTTP Message Signatures with wolfCrypt

Minimal [RFC 9421](https://www.rfc-editor.org/rfc/rfc9421) implementation
using wolfCrypt Ed25519. Covers derived components (`@method`, `@authority`,
`@path`, `@query`), arbitrary headers, timestamp validation, and `keyid`
extraction. Single signature (`sig1`), Ed25519 only.

## Quick start

```sh
# wolfSSL prerequisite
cd wolfssl && ./configure --enable-ed25519 --enable-coding && make && sudo make install

# Build and test
cd http-message-signatures
make
make test        # 17 tests including RFC 9421 B.2.6 vector
./sign_request   # standalone signing example
```

## Client/server demo

```sh
./http_server_verify &   # terminal 1
./http_client_signed     # terminal 2
```

The client sends three requests: a valid signed POST (→ 200), a tampered
request (→ 401), and a valid signed GET (→ 200).

## API

```c
/* Sign — outputs NUL-terminated Signature and Signature-Input header values */
int wc_HttpSig_Sign(
    const char* method, const char* authority,
    const char* path, const char* query,
    const wc_HttpHeader* headers, int headerCount,
    ed25519_key* key, const char* keyId, long created,
    char* sigOut, word32* sigOutSz,
    char* inputOut, word32* inputOutSz);

/* Extract keyid from Signature-Input */
int wc_HttpSig_GetKeyId(
    const char* signatureInput, const char* label,
    char* keyIdOut, word32* keyIdOutSz);

/* Verify — returns 0 on success; maxAgeSec=0 skips timestamp check */
int wc_HttpSig_Verify(
    const char* method, const char* authority,
    const char* path, const char* query,
    const wc_HttpHeader* headers, int headerCount,
    const char* signature, const char* signatureInput,
    const char* label, ed25519_key* pubKey, int maxAgeSec);
```

Typical verify flow:

```
1. wc_HttpSig_GetKeyId(sigInput, label, keyIdBuf, &sz)
2. my_key_store_lookup(keyIdBuf) → pubKey
3. wc_HttpSig_Verify(..., pubKey, maxAgeSec)
```

## Limitations

- **No body protection** — no `content-digest` (RFC 9530); signatures cover
  components and headers only
- **No replay prevention** — `maxAgeSec` limits the window; nonce tracking
  is the caller's responsibility
- **Duplicate headers** — first match wins, no folding
- **Y2038** — `created` uses `long`; overflows on platforms where `long` is
  32-bit (all 32-bit targets and 64-bit Windows/LLP64) after 2038-01-19
- **Canonicalization** — signer and verifier must agree on component
  representation; intermediaries that rewrite headers will break verification

## Files

```
common/wc_http_sig.{c,h}   RFC 9421 sign/verify/getKeyId
common/wc_sf.{c,h}         RFC 8941 structured fields (subset)
sign_request.c              Standalone signing example
http_server_verify.c        Demo server with signature verification
http_client_signed.c        Demo client sending signed requests
test_vectors.c              17 tests including RFC 9421 B.2.6
```

## References

- [RFC 9421 — HTTP Message Signatures](https://www.rfc-editor.org/rfc/rfc9421)
- [RFC 8941 — Structured Field Values for HTTP](https://www.rfc-editor.org/rfc/rfc8941)
- [wolfSSL Ed25519 Documentation](https://www.wolfssl.com/documentation/manuals/wolfssl/group__ED25519.html)
