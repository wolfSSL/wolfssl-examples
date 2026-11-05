# RTL8735B (AmebaPro2) HUK-in-TLS example

In-memory TLS 1.2 ECDHE-ECDSA handshake (client + server in one firmware, no
networking) where the server's certificate ECDSA P-256 signature is produced on
the RTL8735B HW ECDSA engine via the wolfCrypt HUK crypto-callback port. The
ECDHE key agreement and record-layer AES-GCM run in software. Proves
HUK/HW-accelerated server authentication in a real TLS handshake.

A second handshake runs mutual TLS where the client's private key is stored only as a HUK-wrapped blob: the client's CertificateVerify signature is made from the blob, and the plaintext key is never loaded into wolfSSL.

Validated on RTL8735B silicon:

```
== TLS 1.2 ECDHE-ECDSA (server auth on HUK) ==
[PASS] CTX_new (client+server)
[PASS] server cert + key
[PASS] client load_verify (CA)
cipher: ECDHE-ECDSA-AES128-GCM-SHA256
[PASS] handshake + record (server signs on HW)

== TLS 1.2 mutual auth, client key HUK-wrapped ==
[PASS] CTX_new (client+server)
[PASS] server cert + key, client cert required
[PASS] client cert + public key only
-- GCM-wrapped client key --
[PASS] wrap the client private key under the HUK
cipher: ECDHE-ECDSA-AES128-GCM-SHA256
[PASS] mutual handshake + record (client signs from the HUK blob)
client err -180
[PASS] tampered HUK blob fails the handshake
-- ECB-wrapped client key --
[PASS] wrap the client private key under the HUK
cipher: ECDHE-ECDSA-AES128-GCM-SHA256
[PASS] mutual handshake + record (client signs from the HUK blob)
server err -229
[PASS] tampered HUK blob fails the handshake
```

The ECB-wrapped client key needs the ECB support in wolfSSL PR #12056; against older wolfSSL the ECB handshake fails with `BAD_FUNC_ARG` (-170).

## Key design point: use a PK sign callback, NOT a whole-CTX devId

Do NOT `wolfSSL_CTX_SetDevId(ctx, WC_HUK_DEVID)` for a TLS CTX. The HUK device
treats a 32-byte HMAC/AES key as a *HUK seed* (HKDF input), but TLS legitimately
uses 32-byte keys in its own crypto -- e.g. the P-256 ECDHE pre-master secret is
32 bytes and TLS 1.2's PRF computes `master_secret = HMAC-SHA256(pre_master,...)`
with it. Routing that PRF HMAC to the HUK derives the wrong master secret ->
wrong session keys -> the peer's Finished fails (`VERIFY_MAC_ERROR`, -305).

Instead, route ONLY the server's certificate signature to the HUK with a PK ECC
sign callback (`wolfSSL_CTX_SetEccSignCb`). Inside it, sign with a temporary
`ecc_key` whose `devId = WC_HUK_DEVID`, so only that ECDSA sign dispatches to the
HW engine while the PRF/HMAC, record AES, and ECDHE stay in software. See
`huk_ecc_sign_cb` in `main.c`. Requires `HAVE_PK_CALLBACKS`.

## Mutual TLS with a HUK-wrapped client key

`huk_mtls_test` in `main.c` shows a device-bound client key:

1. Provisioning (`huk_wrap_client_key`): the client's P-256 scalar is wrapped under the HUK with a fresh seed, using AES-GCM (recommended, with a fresh IV) or AES-ECB (the STM32 DHUK blob layout, for code shared between the two parts), and the plaintext is scrubbed. The stored blob is the format, seed and wrapped scalar, plus the IV and tag for GCM; only this device can unwrap it. In the example the scalar comes from `certs_test.h` as a stand-in for a key whose certificate was issued at manufacture.
2. Client setup: `wolfSSL_CTX_SetEccSignCb(cctx, huk_wrapped_sign_cb)` first, then `wolfSSL_CTX_use_certificate_buffer` with the client certificate and `wolfSSL_CTX_use_PrivateKey_buffer` with the client's **public** key. With a sign callback set, wolfSSL accepts a public key in place of the private key (`WOLF_PRIVATE_KEY_ID`, enabled automatically by `HAVE_PK_CALLBACKS`) and sends every signature to the callback.
3. Per connection: `wolfSSL_SetEccSignCtx(ssl, &blob)`. The callback builds a `wc_Rtl8735b_EccKey` from the blob, attaches it to a `WC_HUK_DEVID` `ecc_key` through `devCtx`, and calls `wc_ecc_sign_hash`, which unwraps the scalar under the HUK and signs.

The example runs the handshake with both formats, and with a tampered blob of each. A tampered or wrong-device GCM blob fails its unwrap on the client, so nothing is signed. ECB has no integrity check: a tampered blob unwraps to a wrong scalar and signs, and the server rejects the CertificateVerify, so the handshake still fails but only at the peer. An ECB blob leaves the `iv` and `tag` fields of `wc_Rtl8735b_EccKey` NULL/0, which is how the port selects the ECB unwrap, so the callback rejects an unknown format byte rather than falling back to ECB. The unwrapped scalar is in RAM only for the duration of each sign.

## Build

Install this directory at `<SDK>/component/example/wolfcrypt_huk_tls/` and select
it in the RealTek AmebaPro2 FreeRTOS SDK build:

```
cmake .. -DEXAMPLE=wolfcrypt_huk_tls -DWOLFSSL_ROOT=/path/to/wolfssl ...
```

(also copy `main.c` to `<SDK>/project/realtek_amebapro2_v0_example/src/main.c`).
See the `amebapro2-flash` build/flash flow used by the sibling `wolfcrypt_huk`
example.

## Notes

- P-256 test certificates come from wolfSSL's `certs_test.h` (`USE_CERT_BUFFERS_256`:
  `serv_ecc_der_256`, `ecc_key_der_256`, `ca_ecc_cert_der_256`).
- `NO_ASN_TIME` is set (the board has no RTC, so certificate date checks are
  skipped) -- do not use that in production without a real time source.
- `WOLFSSL_USER_IO` + `WOLFSSL_NO_SOCK`: the transport is two in-memory buffers
  via custom `SetIORecv`/`SetIOSend` callbacks; no sockets are used.
- The mutual-auth handshake also uses `cliecc_cert_der_256`, `ecc_clikey_der_256` (wrapped at provisioning only) and `ecc_clikeypub_der_256`. The server trusts the self-signed client certificate directly.
